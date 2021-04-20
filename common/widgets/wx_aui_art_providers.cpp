/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/aui/aui.h>
#include <wx/aui/framemanager.h>
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/settings.h>

#include <kiplatform/ui.h>
#include <widgets/wx_aui_art_providers.h>

/**
 * wxAuiDefaultToolBarArt::DrawButton except with dark-mode awareness based on BITMAP_BUTTON
 * Unfortunately, wx 3.0 does not provide any hooks that would make it possible to do this in a way
 * other than just copy/pasting the upstream implementation and modifying it...
 */
void WX_AUI_TOOLBAR_ART::DrawButton( wxDC& aDc, wxWindow* aWindow, const wxAuiToolBarItem& aItem,
                                     const wxRect& aRect )
{
    bool darkMode   = KIPLATFORM::UI::IsDarkTheme();
    int  textWidth  = 0;
    int  textHeight = 0;

    if( m_flags & wxAUI_TB_TEXT )
    {
        aDc.SetFont( m_font );

        int tx, ty;

        aDc.GetTextExtent( wxT( "ABCDHgj" ), &tx, &textHeight );
        textWidth = 0;
        aDc.GetTextExtent( aItem.GetLabel(), &textWidth, &ty );
    }

    int bmpX = 0, bmpY = 0;
    int textX = 0, textY = 0;

    if( m_textOrientation == wxAUI_TBTOOL_TEXT_BOTTOM )
    {
        bmpX = aRect.x + ( aRect.width / 2 ) - ( aItem.GetBitmap().GetWidth() / 2 );

        bmpY = aRect.y + ( ( aRect.height - textHeight ) / 2 ) - ( aItem.GetBitmap().GetHeight() / 2 );

        textX = aRect.x + ( aRect.width / 2 ) - ( textWidth / 2 ) + 1;
        textY = aRect.y + aRect.height - textHeight - 1;
    }
    else if( m_textOrientation == wxAUI_TBTOOL_TEXT_RIGHT )
    {
        bmpX = aRect.x + 3;

        bmpY = aRect.y + ( aRect.height / 2 ) - ( aItem.GetBitmap().GetHeight() / 2 );

        textX = bmpX + 3 + aItem.GetBitmap().GetWidth();
        textY = aRect.y + ( aRect.height / 2 ) - ( textHeight / 2 );
    }

    if( !( aItem.GetState() & wxAUI_BUTTON_STATE_DISABLED ) )
    {
        if( aItem.GetState() & wxAUI_BUTTON_STATE_PRESSED )
        {
            aDc.SetPen( wxPen( m_highlightColour ) );
            aDc.SetBrush( wxBrush( m_highlightColour.ChangeLightness( darkMode ? 20 : 150 ) ) );
            aDc.DrawRectangle( aRect );
        }
        else if( ( aItem.GetState() & wxAUI_BUTTON_STATE_HOVER ) || aItem.IsSticky() )
        {
            aDc.SetPen( wxPen( m_highlightColour ) );
            aDc.SetBrush( wxBrush( m_highlightColour.ChangeLightness( darkMode  ? 40 : 170 ) ) );

            // draw an even lighter background for checked aItem hovers (since
            // the hover background is the same color as the check background)
            if( aItem.GetState() & wxAUI_BUTTON_STATE_CHECKED )
                aDc.SetBrush( wxBrush( m_highlightColour.ChangeLightness( darkMode ? 50 : 180 ) ) );

            aDc.DrawRectangle( aRect );
        }
        else if( aItem.GetState() & wxAUI_BUTTON_STATE_CHECKED )
        {
            // it's important to put this code in an else statement after the
            // hover, otherwise hovers won't draw properly for checked aItems
            aDc.SetPen( wxPen( m_highlightColour ) );
            aDc.SetBrush( wxBrush( m_highlightColour.ChangeLightness( darkMode  ? 40 : 170 ) ) );
            aDc.DrawRectangle( aRect );
        }
    }

    wxBitmap bmp;

    if( aItem.GetState() & wxAUI_BUTTON_STATE_DISABLED )
        bmp = aItem.GetDisabledBitmap();
    else
        bmp = aItem.GetBitmap();

    if( bmp.IsOk() )
        aDc.DrawBitmap( bmp, bmpX, bmpY, true );

    // set the aItem's text color based on if it is disabled
    aDc.SetTextForeground( *wxBLACK );

    if( aItem.GetState() & wxAUI_BUTTON_STATE_DISABLED )
        aDc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    if( ( m_flags & wxAUI_TB_TEXT ) && !aItem.GetLabel().empty() )
    {
        aDc.DrawText( aItem.GetLabel(), textX, textY );
    }
}


WX_AUI_DOCK_ART::WX_AUI_DOCK_ART() : wxAuiDefaultDockArt()
{
#if defined( _WIN32 )
    #if wxCHECK_VERSION( 3, 1, 0 )
    // Use normal control font, wx likes to use "small"
    m_captionFont = *wxNORMAL_FONT;

    // Increase the box the caption rests in size a bit
    m_captionSize = wxWindow::FromDIP( 20, NULL );
#endif
#endif

    SetColour( wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR,
               wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
    SetColour( wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR,
               wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );

    // Turn off the ridiculous looking gradient
    m_gradientType = wxAUI_GRADIENT_NONE;
}
