/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011-2012 Wayne Stambaugh <stambaughw@gmail.com>
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

/**
 * @file msgpanel.h
 * @brief Message panel definition file.
 */

#pragma once

#include <optional>
#include <vector>

#include <gal/color4d.h>

#include <wx/window.h>
#include <wx/panel.h>

using KIGFX::COLOR4D;

#define MSG_PANEL_DEFAULT_PAD      6  ///< The default number of spaces between each text string.


class EDA_MSG_PANEL;
class KIID;


/**
 * #EDA_MSG_PANEL items for displaying messages.
 */
class MSG_PANEL_ITEM
{
public:
    MSG_PANEL_ITEM( const wxString& aUpperText, const wxString& aLowerText,
                    int aPadding = MSG_PANEL_DEFAULT_PAD ) :
            m_UpperText( aUpperText ),
            m_LowerText( aLowerText ),
            m_Padding( aPadding )
    {
        m_X = 0;
        m_UpperY = 0;
        m_LowerY = 0;
    }

    MSG_PANEL_ITEM() :
            m_Padding( MSG_PANEL_DEFAULT_PAD )

    {
        m_X = 0;
        m_UpperY = 0;
        m_LowerY = 0;
    }

    void SetUpperText( const wxString& aUpperText ) { m_UpperText = aUpperText; }
    const wxString& GetUpperText() const { return m_UpperText; }

    void SetLowerText( const wxString& aLowerText )  { m_LowerText = aLowerText; }
    const wxString& GetLowerText() const { return m_LowerText; }

    void SetPadding( int aPadding )  { m_Padding = aPadding; }
    int GetPadding() const { return m_Padding; }

private:
    friend class EDA_MSG_PANEL;

    int         m_X;
    int         m_UpperY;
    int         m_LowerY;
    wxString    m_UpperText;
    wxString    m_LowerText;
    int         m_Padding;
};


/**
 * A panel to display various information messages.
 */
class EDA_MSG_PANEL : public wxPanel
{
public:
    EDA_MSG_PANEL( wxWindow* aParent, int aId,
                   const wxPoint& aPosition, const wxSize& aSize,
                   long style=wxTAB_TRAVERSAL, const wxString& name=wxPanelNameStr );
    ~EDA_MSG_PANEL();

    void OnPaint( wxPaintEvent& aEvent );
    void OnDPIChanged( wxDPIChangedEvent& aEvent );
    void OnSize( wxSizeEvent& aEvent );
    void EraseMsgBox();

    wxSize DoGetBestSize() const override;
    wxSize DoGetBestClientSize() const override;

   /**
    * Append a message to the message panel.
    *
    * This method automatically adjusts for the width of the text string.
    * Making consecutive calls to AppendMessage will append each message
    * to the right of the last message.  This message is not compatible
    * with Affiche_1_Parametre.
    *
    * @param aUpperText The message upper text.
    * @param aLowerText The message lower text.
    * @param aPadding Number of spaces to pad between messages (default = 4).
    */
    void AppendMessage( const wxString& aUpperText, const wxString& aLowerText, int aPadding = 6 );

   /**
    * Append \a aMessageItem to the message panel.
    *
    * @param aMessageItem is a reference to an #MSG_PANEL_ITEM containing the message to
    *                     append to the panel.
    */
    void AppendMessage( const MSG_PANEL_ITEM& aMessageItem )
    {
        AppendMessage( aMessageItem.GetUpperText(), aMessageItem.GetLowerText(),
                       aMessageItem.GetPadding() );
    }

    DECLARE_EVENT_TABLE()

protected:
    void updateFontSize();

    void rebuildItems();

    void updateItemPos( MSG_PANEL_ITEM& aItem );

    void showItem( wxDC& dc, const MSG_PANEL_ITEM& aItem );

    void erase( wxDC* DC );

protected:
    std::vector<MSG_PANEL_ITEM> m_Items;
    int                         m_last_x;      ///< the last used x coordinate
    wxSize                      m_fontSize;
};


/**
 * Get a formatted UUID string for display in the message panel,
 * according to the current advanced configuration setting.
 *
 * This will be std::nullopt if the configuration setting disables UUID display.
 */
std::optional<wxString> GetMsgPanelDisplayUuid( const KIID& aKiid );
