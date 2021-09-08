/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file msgpanel.cpp
 * @brief Message panel implementation file.
 */

#include <widgets/msgpanel.h>

#include <wx/dcscreen.h>
#include <wx/dcclient.h>
#include <wx/settings.h>
#include <wx/toplevel.h>

#include <widgets/ui_common.h>


BEGIN_EVENT_TABLE( EDA_MSG_PANEL, wxPanel )
    EVT_PAINT( EDA_MSG_PANEL::OnPaint )
END_EVENT_TABLE()


EDA_MSG_PANEL::EDA_MSG_PANEL( wxWindow* aParent, int aId,
                              const wxPoint& aPosition, const wxSize& aSize,
                              long style, const wxString &name ) :
    wxPanel( aParent, aId, aPosition, aSize, style, name )
{
    SetFont( KIUI::GetStatusFont() );
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    // informs wx not to paint the background itself as we will paint it later in erase()
    SetBackgroundStyle( wxBG_STYLE_PAINT );

    m_last_x = 0;

    m_fontSize = GetTextExtent( wxT( "W" ) );
}


EDA_MSG_PANEL::~EDA_MSG_PANEL()
{
}


int EDA_MSG_PANEL::GetRequiredHeight()
{
    wxSize     fontSizeInPixels;
    wxScreenDC dc;

    dc.SetFont( KIUI::GetStatusFont() );
    dc.GetTextExtent( wxT( "W" ), &fontSizeInPixels.x, &fontSizeInPixels.y );

    // make space for two rows of text plus a number of pixels between them.
    return 2 * fontSizeInPixels.y + 0;
}


void EDA_MSG_PANEL::OnPaint( wxPaintEvent& aEvent )
{
    wxPaintDC dc( this );

    erase( &dc );

    dc.SetBackground( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    dc.SetBackgroundMode( wxSOLID );
    dc.SetTextBackground( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    dc.SetFont( KIUI::GetStatusFont() );

    for( const MSG_PANEL_ITEM& item : m_Items )
        showItem( dc, item );

    aEvent.Skip();
}


void EDA_MSG_PANEL::AppendMessage( const wxString& aUpperText, const wxString& aLowerText,
                                   int aPadding )
{
    wxString    text;
    wxSize      drawSize = GetClientSize();

    text = ( aUpperText.Len() > aLowerText.Len() ) ? aUpperText : aLowerText;
    text.Append( ' ', aPadding );

    MSG_PANEL_ITEM item;

    /* Don't put the first message a window client position 0.  Offset by
     * one 'W' character width. */
    if( m_last_x == 0 )
        m_last_x = m_fontSize.x;

    item.m_X = m_last_x;

    item.m_UpperY = ( drawSize.y / 2 ) - m_fontSize.y;
    item.m_LowerY = drawSize.y - m_fontSize.y;

    item.m_UpperText = aUpperText;
    item.m_LowerText = aLowerText;
    m_Items.push_back( item );
    m_last_x += GetTextExtent( text ).x;

    // Add an extra space between texts for a better look:
    m_last_x += m_fontSize.x;

    Refresh();
}


void EDA_MSG_PANEL::SetMessage( int aXPosition, const wxString& aUpperText,
                                const wxString& aLowerText )
{
    wxPoint pos;
    wxSize drawSize = GetClientSize();

    if( aXPosition >= 0 )
        m_last_x = pos.x = aXPosition * (m_fontSize.x + 2);
    else
        pos.x = m_last_x;

    MSG_PANEL_ITEM item;

    item.m_X = pos.x;

    item.m_UpperY = (drawSize.y / 2) - m_fontSize.y;
    item.m_LowerY = drawSize.y - m_fontSize.y;

    item.m_UpperText = aUpperText;
    item.m_LowerText = aLowerText;

    int ndx;

    // update the vector, which is sorted by m_X
    int limit = m_Items.size();

    for( ndx=0;  ndx<limit;  ++ndx )
    {
        // replace any item with same X
        if( m_Items[ndx].m_X == item.m_X )
        {
            m_Items[ndx] = item;
            break;
        }

        if( m_Items[ndx].m_X > item.m_X )
        {
            m_Items.insert( m_Items.begin() + ndx, item );
            break;
        }
    }

    if( ndx == limit )        // mutually exclusive with two above if tests
        m_Items.push_back( item );

    Refresh();
}


void EDA_MSG_PANEL::showItem( wxDC& aDC, const MSG_PANEL_ITEM& aItem )
{
    COLOR4D color;

    // Change the text to a disabled color when the window isn't active
    wxTopLevelWindow* tlw = dynamic_cast<wxTopLevelWindow*>( wxGetTopLevelParent( this ) );

    if( tlw && !tlw->IsActive() )
        color = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );
    else
        color = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

    aDC.SetTextForeground( color.ToColour() );

    if( !aItem.m_UpperText.IsEmpty() )
        aDC.DrawText( aItem.m_UpperText, aItem.m_X, aItem.m_UpperY );

    if( !aItem.m_LowerText.IsEmpty() )
        aDC.DrawText( aItem.m_LowerText, aItem.m_X, aItem.m_LowerY );
}


void EDA_MSG_PANEL::EraseMsgBox()
{
   m_Items.clear();
   m_last_x = 0;
   Refresh();
}


void EDA_MSG_PANEL::erase( wxDC* aDC )
{
    wxPen   pen;
    wxBrush brush;

    wxSize  size  = GetClientSize();
    wxColour color = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );

    pen.SetColour( color );

    brush.SetColour( color );
    brush.SetStyle( wxBRUSHSTYLE_SOLID );

    aDC->SetPen( pen );
    aDC->SetBrush( brush );
    aDC->DrawRectangle( 0, 0, size.x, size.y );
}
