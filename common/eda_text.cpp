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
#include <gr_text.h>
#include <eda_rect.h>
#include <macros.h>
#include <trigo.h>               // RotatePoint

#include <basic_gal.h>
#include <base_units.h>
#include <convert_to_biu.h>

// Sadly we store the orientation of hierarchical and global labels using a different
// int encoding than that for local labels:
//                   Global      Local
// Left justified      0           2
// Up                  1           3
// Right justified     2           0
// Down                3           1
int EDA_TEXT::MapOrientation( KICAD_T labelType, int aOrientation )
{
    if( labelType == SCH_LABEL_T )
        return aOrientation;

    switch( aOrientation )
    {
    case 0: return 2;
    case 2: return 0;
    default: return aOrientation;
    }
}


EDA_TEXT_HJUSTIFY_T EDA_TEXT::MapHorizJustify( int aHorizJustify )
{
    wxASSERT( aHorizJustify >= GR_TEXT_HJUSTIFY_LEFT && aHorizJustify <= GR_TEXT_HJUSTIFY_RIGHT );

    if( aHorizJustify > GR_TEXT_HJUSTIFY_RIGHT )
        return GR_TEXT_HJUSTIFY_RIGHT;

    if( aHorizJustify < GR_TEXT_HJUSTIFY_LEFT )
        return GR_TEXT_HJUSTIFY_LEFT;

    return (EDA_TEXT_HJUSTIFY_T) aHorizJustify;
}


EDA_TEXT_VJUSTIFY_T EDA_TEXT::MapVertJustify( int aVertJustify )
{
    wxASSERT( aVertJustify >= GR_TEXT_VJUSTIFY_TOP && aVertJustify <= GR_TEXT_VJUSTIFY_BOTTOM );

    if( aVertJustify > GR_TEXT_VJUSTIFY_BOTTOM )
        return GR_TEXT_VJUSTIFY_BOTTOM;

    if( aVertJustify < GR_TEXT_VJUSTIFY_TOP )
        return GR_TEXT_VJUSTIFY_TOP;

    return (EDA_TEXT_VJUSTIFY_T) aVertJustify;
}


EDA_TEXT::EDA_TEXT( const wxString& text ) :
        m_text( text ),
        m_e( 1<<TE_VISIBLE )
{
    int sz = Mils2iu( DEFAULT_SIZE_TEXT );
    SetTextSize( wxSize( sz, sz ) );
    m_shown_text = UnescapeString( text );
}


EDA_TEXT::EDA_TEXT( const EDA_TEXT& aText ) :
        m_text( aText.m_text ),
        m_e( aText.m_e )
{
    m_shown_text = UnescapeString( m_text );
}


EDA_TEXT::~EDA_TEXT()
{
}


void EDA_TEXT::SetText( const wxString& aText )
{
    m_text = aText;
    m_shown_text = UnescapeString( aText );
}


void EDA_TEXT::SetEffects( const EDA_TEXT& aSrc )
{
    m_e = aSrc.m_e;
}


void EDA_TEXT::SwapText( EDA_TEXT& aTradingPartner )
{
    std::swap( m_text, aTradingPartner.m_text );
    std::swap( m_shown_text, aTradingPartner.m_shown_text );
}


void EDA_TEXT::SwapEffects( EDA_TEXT& aTradingPartner )
{
    std::swap( m_e, aTradingPartner.m_e );
}


bool EDA_TEXT::Replace( wxFindReplaceData& aSearchData )
{
    return EDA_ITEM::Replace( aSearchData, m_text );
}


int EDA_TEXT::LenSize( const wxString& aLine, int aThickness ) const
{
    basic_gal.SetFontItalic( IsItalic() );
    basic_gal.SetFontBold( IsBold() );
    basic_gal.SetLineWidth( (float) aThickness );
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


int EDA_TEXT::GetInterline() const
{
    return KiROUND( KIGFX::STROKE_FONT::GetInterline( GetTextHeight() ) );
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
    int dy = GetInterline();

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
            yoffset = linecount * GetInterline();
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

    return rect.Intersects( GetTextBox( -1 ), GetTextAngle() );
}


void EDA_TEXT::Print( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor, EDA_DRAW_MODE_T aFillMode )
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
            printOneLineOfText( aDC, aOffset, aColor, aFillMode, txt, positions[ii] );
        }
    }
    else
        printOneLineOfText( aDC, aOffset, aColor, aFillMode, GetShownText(), GetTextPos() );
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

void EDA_TEXT::printOneLineOfText( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor,
                                   EDA_DRAW_MODE_T aFillMode,  const wxString& aText,
                                   const wxPoint &aPos )
{
    int width = GetThickness();

    if( aFillMode == SKETCH )
        width = -width;

    wxSize size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    GRText( aDC, aOffset + aPos, aColor, aText, GetTextAngle(), size, GetHorizJustify(),
            GetVertJustify(), width, IsItalic(), IsBold() );
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
    return (  IsVisible()
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
{
#ifndef GERBVIEW        // Gerbview does not use EDA_TEXT::Format
                        // and does not define FormatInternalUnits, used here
                        // however this function should exist

	aFormatter->Print( aNestLevel + 1, "(effects" );

	// Text size
	aFormatter->Print( 0, " (font" );

	aFormatter->Print( 0, " (size %s %s)",
					   FormatInternalUnits( GetTextHeight() ).c_str(),
					   FormatInternalUnits( GetTextWidth() ).c_str() );

	if( GetThickness() )
		aFormatter->Print( 0, " (thickness %s)", FormatInternalUnits( GetThickness() ).c_str() );

	if( IsBold() )
		aFormatter->Print( 0, " bold" );

	if( IsItalic() )
		aFormatter->Print( 0, " italic" );

	aFormatter->Print( 0, ")"); // (font

	if( IsMirrored() ||
	    GetHorizJustify() != GR_TEXT_HJUSTIFY_CENTER ||
	    GetVertJustify() != GR_TEXT_VJUSTIFY_CENTER )
	{
		aFormatter->Print( 0, " (justify");

		if( GetHorizJustify() != GR_TEXT_HJUSTIFY_CENTER )
			aFormatter->Print( 0, (GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT) ? " left" : " right" );

		if( GetVertJustify() != GR_TEXT_VJUSTIFY_CENTER )
			aFormatter->Print( 0, (GetVertJustify() == GR_TEXT_VJUSTIFY_TOP) ? " top" : " bottom" );

		if( IsMirrored() )
			aFormatter->Print( 0, " mirror" );
		aFormatter->Print( 0, ")" ); // (justify
	}

	if( !(aControlBits & CTL_OMIT_HIDE) && !IsVisible() )
		aFormatter->Print( 0, " hide" );

	aFormatter->Print( 0, ")\n" ); // (justify

#endif
}

// Convert the text shape to a list of segment
// each segment is stored as 2 wxPoints: its starting point and its ending point
// we are using GRText to create the segments and therefore a call-back function is needed

// This is a call back function, used by GRText to put each segment in buffer
static void addTextSegmToBuffer( int x0, int y0, int xf, int yf, void* aData )
{
    std::vector<wxPoint>* cornerBuffer = static_cast<std::vector<wxPoint>*>( aData );
    cornerBuffer->push_back( wxPoint( x0, y0 ) );
    cornerBuffer->push_back( wxPoint( xf, yf ) );
}


void EDA_TEXT::TransformTextShapeToSegmentList( std::vector<wxPoint>& aCornerBuffer ) const
{
    wxSize size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    COLOR4D color = COLOR4D::BLACK;  // not actually used, but needed by GRText

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
            GRText( NULL, positions[ii], color, txt, GetTextAngle(), size, GetHorizJustify(),
                    GetVertJustify(), GetThickness(), IsItalic(), true, addTextSegmToBuffer,
                    &aCornerBuffer );
        }
    }
    else
    {
        GRText( NULL, GetTextPos(), color, GetText(), GetTextAngle(), size, GetHorizJustify(),
                GetVertJustify(), GetThickness(), IsItalic(), true, addTextSegmToBuffer,
                &aCornerBuffer );
    }
}
