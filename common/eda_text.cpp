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
#include <macros.h>              // MAX
#include <trigo.h>               // RotatePoint
#include <class_drawpanel.h>     // EDA_DRAW_PANEL


// Conversion to application internal units defined at build time.
#if defined( PCBNEW )
#include <class_board_item.h>
#define MILS_TO_IU( x )     ( x * IU_PER_MILS );
#elif defined( EESCHEMA )
#include <sch_item_struct.h>
#define MILS_TO_IU( x )     ( x )
#else
#error "Cannot resolve units formatting due to no definition of EESCHEMA or PCBNEW."
#endif


EDA_TEXT::EDA_TEXT( const wxString& text )
{
    m_Size.x    = m_Size.y = MILS_TO_IU( DEFAULT_SIZE_TEXT );  // Width and height of font.
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
    return ReturnGraphicTextWidth( aLine, m_Size.x, m_Italic, m_Bold );
}


EDA_RECT EDA_TEXT::GetTextBox( int aLine, int aThickness, bool aInvertY ) const
{
    EDA_RECT       rect;
    wxPoint        pos;
    wxArrayString* list = NULL;
    wxString       text = m_Text;
    int            thickness = ( aThickness < 0 ) ? m_Thickness : aThickness;

    if( m_MultilineAllowed )
    {
        list = wxStringSplit( m_Text, '\n' );

        if ( list->GetCount() )     // GetCount() == 0 for void strings
        {
            if( aLine >= 0 && (aLine < (int)list->GetCount()) )
                text = list->Item( aLine );
            else
                text = list->Item( 0 );
        }
    }

    // calculate the H and V size
    int    dx = LenSize( text );
    int    dy = GetInterline();

    /* Creates bounding box (rectangle) for an horizontal text */
    wxSize textsize = wxSize( dx, dy );

    if( aInvertY )
        rect.SetOrigin( m_Pos.x, -m_Pos.y );
    else
        rect.SetOrigin( m_Pos );

    // extra dy interval for letters like j and y and ]
    int extra_dy = dy - m_Size.y;
    rect.Move( wxPoint( 0, -extra_dy / 2 ) ); // move origin by the half extra interval

    // for multiline texts and aLine < 0, merge all rectangles
    if( m_MultilineAllowed && list && aLine < 0 )
    {
        for( unsigned ii = 1; ii < list->GetCount(); ii++ )
        {
            text = list->Item( ii );
            dx   = LenSize( text );
            textsize.x  = MAX( textsize.x, dx );
            textsize.y += dy;
        }
    }

    delete list;

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
        rect.SetY( rect.GetY() - (dy / 2) );
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        rect.SetY( rect.GetY() - dy );
        break;
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


void EDA_TEXT::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                     EDA_COLOR_T aColor, int aDrawMode,
                     EDA_DRAW_MODE_T aFillMode, EDA_COLOR_T aAnchor_color )
{
    if( m_MultilineAllowed )
    {
        wxPoint        pos  = m_Pos;
        wxArrayString* list = wxStringSplit( m_Text, '\n' );
        wxPoint        offset;

        offset.y = GetInterline();

        RotatePoint( &offset, m_Orient );

        for( unsigned i = 0; i<list->Count(); i++ )
        {
            wxString txt = list->Item( i );
            DrawOneLineOfText( aPanel,
                               aDC,
                               aOffset,
                               aColor,
                               aDrawMode,
                               aFillMode,
                               i ?  UNSPECIFIED : aAnchor_color,
                               txt,
                               pos );
            pos += offset;
        }

        delete (list);
    }
    else
        DrawOneLineOfText( aPanel,
                           aDC,
                           aOffset,
                           aColor,
                           aDrawMode,
                           aFillMode,
                           aAnchor_color,
                           m_Text,
                           m_Pos );
}


void EDA_TEXT::DrawOneLineOfText( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                  const wxPoint& aOffset, EDA_COLOR_T aColor,
                                  int aDrawMode, EDA_DRAW_MODE_T aFillMode,
                                  EDA_COLOR_T aAnchor_color,
                                  wxString& aText, wxPoint aPos )
{
    int width = m_Thickness;

    if( aFillMode == LINE )
        width = 0;

    if( aDrawMode != -1 )
        GRSetDrawMode( aDC, aDrawMode );

    /* Draw text anchor, if allowed */
    if( aAnchor_color != UNSPECIFIED )
    {

        int anchor_size = aDC->DeviceToLogicalXRel( 2 );

        aAnchor_color = (EDA_COLOR_T) ( aAnchor_color & MASKCOLOR );

        int cX = aPos.x + aOffset.x;
        int cY = aPos.y + aOffset.y;

        GRLine( aPanel->GetClipBox(), aDC, cX - anchor_size, cY,
                cX + anchor_size, cY, 0, aAnchor_color );

        GRLine( aPanel->GetClipBox(), aDC, cX, cY - anchor_size,
                cX, cY + anchor_size, 0, aAnchor_color );
    }

    if( aFillMode == SKETCH )
        width = -width;

    wxSize size = m_Size;

    if( m_Mirror )
        size.x = -size.x;

    DrawGraphicText( aPanel, aDC, aOffset + aPos, aColor, aText, m_Orient, size,
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
    return (  ( m_Size.x == DEFAULT_SIZE_TEXT )
           && ( m_Size.y == DEFAULT_SIZE_TEXT )
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
    if( !IsDefaultFormatting() )
    {
        aFormatter->Print( aNestLevel+1, "(effects\n" );

        if( ( m_Size.x != DEFAULT_SIZE_TEXT ) || ( m_Size.y != DEFAULT_SIZE_TEXT ) || m_Bold
          || m_Italic )
        {
            aFormatter->Print( aNestLevel+2, "(font" );

            // Add font support here at some point in the future.

            if( ( m_Size.x != DEFAULT_SIZE_TEXT ) || ( m_Size.y != DEFAULT_SIZE_TEXT ) )
                aFormatter->Print( 0, " (size %s %s)", FMT_IU( m_Size.GetHeight() ).c_str(),
                                   FMT_IU( m_Size.GetWidth() ).c_str() );

            if( m_Thickness != 0 )
                aFormatter->Print( 0, " (thickness %s)", FMT_IU( m_Thickness ).c_str() );

            if( m_Bold )
                aFormatter->Print( 0, " bold" );

            if( m_Bold )
                aFormatter->Print( 0, " italic" );

            aFormatter->Print( 0, ")\n");
        }

        if( m_Mirror || ( m_HJustify != GR_TEXT_HJUSTIFY_CENTER )
          || ( m_VJustify != GR_TEXT_VJUSTIFY_CENTER ) )
        {
            aFormatter->Print( aNestLevel+2, "(justify");

            if( m_HJustify != GR_TEXT_HJUSTIFY_CENTER )
                aFormatter->Print( 0, (m_HJustify == GR_TEXT_HJUSTIFY_LEFT) ? " left" : " right" );

            if( m_VJustify != GR_TEXT_VJUSTIFY_CENTER )
                aFormatter->Print( 0, (m_VJustify == GR_TEXT_VJUSTIFY_TOP) ? " top" : " bottom" );

            if( m_Mirror )
                aFormatter->Print( 0, " mirror" );

            aFormatter->Print( 0, ")\n" );
        }

        // As of now the only place this is used is in Eeschema to hide or show the text.
        if( m_Attributs )
            aFormatter->Print( aNestLevel+2, "hide\n" );

        aFormatter->Print( aNestLevel+1, ")\n" );
    }
}
