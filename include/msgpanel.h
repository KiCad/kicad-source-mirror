/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011-2012 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file msgpanel.h
 * @brief Message panel definition file.
 */

#ifndef  _MSGPANEL_H_
#define  _MSGPANEL_H_


#include <colors.h>

#include <wx/window.h>

#include <vector>


#define MSG_PANEL_DEFAULT_PAD      6  ///< The default number of spaces between each text string.


class EDA_MSG_PANEL;


/**
 * Class EDA_MSG_ITEM
 * is used EDA_MSG_PANEL as the item type for displaying messages.
 */
class MSG_PANEL_ITEM
{
    int         m_X;
    int         m_UpperY;
    int         m_LowerY;
    wxString    m_UpperText;
    wxString    m_LowerText;
    EDA_COLOR_T m_Color;
    int         m_Pad;

    friend class EDA_MSG_PANEL;

public:
    MSG_PANEL_ITEM( const wxString& aUpperText, const wxString& aLowerText, EDA_COLOR_T aColor,
                    int aPad = MSG_PANEL_DEFAULT_PAD ) :
        m_UpperText( aUpperText ),
        m_LowerText( aLowerText ),
        m_Color( aColor ),
        m_Pad( aPad )
    {
    }

    MSG_PANEL_ITEM() :
        m_Pad( MSG_PANEL_DEFAULT_PAD )
    {
    }

    void SetUpperText( const wxString& aUpperText ) { m_UpperText = aUpperText; }
    const wxString& GetUpperText() const { return m_UpperText; }

    void SetLowerText( const wxString& aLowerText )  { m_LowerText = aLowerText; }
    const wxString& GetLowerText() const { return m_LowerText; }

    void SetColor( EDA_COLOR_T aColor ) { m_Color = aColor; }
    EDA_COLOR_T GetColor() const { return m_Color; }

    void SetPadding( int aPad )  { m_Pad = aPad; }
    int GetPadding() const { return m_Pad; }
};


typedef std::vector<MSG_PANEL_ITEM>      MSG_PANEL_ITEMS;
typedef MSG_PANEL_ITEMS::iterator        MSG_PANEL_ITEMS_ITER;
typedef MSG_PANEL_ITEMS::const_iterator  MSG_PANEL_ITEMS_CITER;


/**
 * class EDA_MSG_PANEL
 * is a panel to display various information messages.
 */
class EDA_MSG_PANEL : public wxPanel
{
protected:
    MSG_PANEL_ITEMS        m_Items;
    int                    m_last_x;      ///< the last used x coordinate
    wxSize                 m_fontSize;

    void showItem( wxDC& dc, const MSG_PANEL_ITEM& aItem );

    void erase( wxDC* DC );

    /**
     * Function getFontSize
     * computes the height and width of a 'W' in the system font.
     */
    static wxSize computeFontSize();

    /**
     * Calculate the width and height of a text string using the system UI font.
     */
    wxSize computeTextSize( const wxString& text ) const;

public:
    EDA_MSG_PANEL( wxWindow* aParent, int aId,
                   const wxPoint& aPosition, const wxSize& aSize,
                   long style=wxTAB_TRAVERSAL, const wxString &name=wxPanelNameStr);
    ~EDA_MSG_PANEL();

    /**
     * Function GetRequiredHeight
     * returns the required height (in pixels) of a EDA_MSG_PANEL.  This takes
     * into consideration the system gui font, wxSYS_DEFAULT_GUI_FONT.
     */
    static int GetRequiredHeight();

    void OnPaint( wxPaintEvent& aEvent );
    void EraseMsgBox();

    /**
     * Function SetMessage
     * sets a message at \a aXPosition to \a aUpperText and \a aLowerText in the message panel.
     *
     * @param aXPosition The horizontal position to display the message or less than zero
     *                   to set the message using the last message position.
     * @param aUpperText The text to be displayed in top line.
     * @param aLowerText The text to be displayed in bottom line.
     * @param aColor Color of the text to display.
     */
    void SetMessage( int aXPosition, const wxString& aUpperText,
                     const wxString& aLowerText, EDA_COLOR_T aColor );

   /**
    * Function AppendMessage
    * appends a message to the message panel.
    *
    * This method automatically adjusts for the width of the text string.
    * Making consecutive calls to AppendMessage will append each message
    * to the right of the last message.  This message is not compatible
    * with Affiche_1_Parametre.
    *
    * @param aUpperText The message upper text.
    * @param aLowerText The message lower text.
    * @param aColor A color ID from the KiCad color list (see colors.h).
    * @param aPad Number of spaces to pad between messages (default = 4).
    */
    void AppendMessage( const wxString& aUpperText, const wxString& aLowerText,
                        EDA_COLOR_T aColor, int aPad = 6 );

   /**
    * Function AppendMessage
    * appends \a aMessageItem to the message panel.
    *
    * @param aMessageItem is a reference to an #MSG_PANEL_ITEM containing the message to
    *                     append to the panel.
    */
    void AppendMessage( const MSG_PANEL_ITEM& aMessageItem )
    {
        AppendMessage( aMessageItem.GetUpperText(), aMessageItem.GetLowerText(),
                       aMessageItem.GetColor(), aMessageItem.GetPadding() );
    }

    DECLARE_EVENT_TABLE()
};


#endif    // _MSGPANEL_H_
