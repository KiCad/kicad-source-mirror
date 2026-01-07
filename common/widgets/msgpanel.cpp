/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/msgpanel.h>

#include <wx/dcscreen.h>
#include <wx/dcclient.h>
#include <wx/settings.h>
#include <wx/toplevel.h>

#include <advanced_config.h>
#include <kiid.h>

#include <widgets/ui_common.h>


BEGIN_EVENT_TABLE( EDA_MSG_PANEL, wxPanel )
    EVT_DPI_CHANGED( EDA_MSG_PANEL::OnDPIChanged )
    EVT_SIZE( EDA_MSG_PANEL::OnSize )
    EVT_PAINT( EDA_MSG_PANEL::OnPaint )
END_EVENT_TABLE()


EDA_MSG_PANEL::EDA_MSG_PANEL( wxWindow* aParent, int aId, const wxPoint& aPosition,
                              const wxSize& aSize, long style, const wxString &name ) :
    wxPanel( aParent, aId, aPosition, aSize, style, name )
{
    SetFont( KIUI::GetStatusFont( this ) );
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    // informs wx not to paint the background itself as we will paint it later in erase()
    SetBackgroundStyle( wxBG_STYLE_PAINT );

    m_last_x = 0;

    updateFontSize();

    InvalidateBestSize();
}


EDA_MSG_PANEL::~EDA_MSG_PANEL()
{
}


void EDA_MSG_PANEL::updateFontSize()
{
    wxFont font = KIUI::GetControlFont( this );
    GetTextExtent( wxT( "W" ), &m_fontSize.x, &m_fontSize.y, 0, 0, &font );
}


wxSize EDA_MSG_PANEL::DoGetBestSize() const
{
    return wxSize( wxDefaultCoord, 2 * m_fontSize.y + 0 );
}


wxSize EDA_MSG_PANEL::DoGetBestClientSize() const
{
    return wxPanel::DoGetBestClientSize();
}


void EDA_MSG_PANEL::OnDPIChanged( wxDPIChangedEvent& aEvent )
{
    updateFontSize();
    InvalidateBestSize();

    aEvent.Skip();
}


void EDA_MSG_PANEL::rebuildItems()
{
    m_last_x = 0;

    for( MSG_PANEL_ITEM& item : m_Items )
        updateItemPos( item );
}


void EDA_MSG_PANEL::OnSize( wxSizeEvent& aEvent )
{
    rebuildItems();
}


void EDA_MSG_PANEL::OnPaint( wxPaintEvent& aEvent )
{
    wxPaintDC dc( this );

    erase( &dc );

    dc.SetBackground( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    dc.SetBackgroundMode( wxSOLID );
    dc.SetTextBackground( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    dc.SetFont( KIUI::GetControlFont( this ) );

    for( const MSG_PANEL_ITEM& item : m_Items )
        showItem( dc, item );

    aEvent.Skip();
}


void EDA_MSG_PANEL::updateItemPos( MSG_PANEL_ITEM& item )
{
    wxString text;
    wxString upperText = item.GetUpperText();
    wxString lowerText = item.GetLowerText();
    wxSize   drawSize = GetClientSize();

    text = ( upperText.Len() > lowerText.Len() ) ? upperText : lowerText;
    text.Append( ' ', item.GetPadding() );

    /* Don't put the first message a window client position 0.  Offset by
     * one 'W' character width. */
    if( m_last_x == 0 )
        m_last_x = m_fontSize.x;

    item.m_X = m_last_x;

    item.m_UpperY = ( drawSize.y / 2 ) - m_fontSize.y;
    item.m_LowerY = drawSize.y - m_fontSize.y;

    m_last_x += GetTextExtent( text ).x;

    // Add an extra space between texts for a better look:
    m_last_x += m_fontSize.x;
}


void EDA_MSG_PANEL::AppendMessage( const wxString& aUpperText, const wxString& aLowerText, int aPadding )
{
    MSG_PANEL_ITEM item;

    item.m_UpperText = aUpperText;
    item.m_LowerText = aLowerText;

    updateItemPos( item );
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


std::optional<wxString> GetMsgPanelDisplayUuid( const KIID& aKiid )
{
    const static int        showUuids = ADVANCED_CFG::GetCfg().m_MsgPanelShowUuids;
    std::optional<wxString> uuid;

    if( showUuids > 0 )
    {
        uuid = aKiid.AsString();

        if( showUuids == 2 )
            uuid = uuid->SubString( 0, 7 );
    }

    return uuid;
}
