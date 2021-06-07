/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee dot org>
 * Copyright (C) 2020-2021 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef BITMAP_BUTTON_H_
#define BITMAP_BUTTON_H_

#include <wx/bitmap.h>
#include <wx/panel.h>


/**
 * A bitmap button widget that behaves like an AUI toolbar item's button when it is drawn.
 * Specifically:
 *     * It has no border
 *     * It has a rectangle highlight when the mouse is hovering/pressed
 *     * It has the ability to be checked/toggled
 */
class BITMAP_BUTTON : public wxPanel
{
public:
    BITMAP_BUTTON( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos = wxDefaultPosition,
                   const wxSize& aSize = wxDefaultSize,
                   int aStyles = wxBORDER_NONE | wxTAB_TRAVERSAL );

    ~BITMAP_BUTTON();

    /**
     * Set the amount of padding present on each side of the bitmap.
     *
     * @param aPadding is the amount in px of padding for each side.
     */
    void SetPadding( int aPadding );

    /**
     * Set the bitmap shown when the button is enabled.
     *
     * @param aBmp is the enabled bitmap.
     */
    void SetBitmap( const wxBitmap& aBmp );

    /**
     * Set the bitmap shown when the button is disabled.
     *
     * @param aBmp is the disabled bitmap.
     */
    void SetDisabledBitmap( const wxBitmap& aBmp );

    /**
     * Enable the button.
     *
     * @param aEnable is true to enable, false to disable.
     */
    bool Enable( bool aEnable = true ) override;

    /**
     * Check the control. This is the equivalent to toggling a toolbar button.
     *
     * @param aCheck is true to check, false to uncheck.
     */
    void Check( bool aCheck = true );

    /**
     * Accept mouse-up as click even if mouse-down happened outside of the control
     *
     * @param aAcceptDragIn is true to allow drag in, false to ignore lone mouse-up events
     */
    void AcceptDragInAsClick( bool aAcceptDragIn = true );

protected:
    void OnMouseLeave( wxEvent& aEvent );
    void OnMouseEnter( wxEvent& aEvent );
    void OnKillFocus( wxEvent& aEvent );
    void OnSetFocus( wxEvent& aEvent );
    void OnLeftButtonUp( wxMouseEvent& aEvent );
    void OnLeftButtonDown( wxMouseEvent& aEvent );
    void OnPaint( wxPaintEvent& aEvent );

    void setFlag( int aFlag )
    {
        m_buttonState |= aFlag;
    }

    void clearFlag( int aFlag )
    {
        m_buttonState &= ~aFlag;
    }

    bool hasFlag( int aFlag )
    {
        return m_buttonState & aFlag;
    }

private:
    ///< Bitmap shown when button is enabled
    wxBitmap  m_normalBitmap;

    ///< Bitmap shown when button is disabled
    wxBitmap  m_disabledBitmap;

    ///< Current state of the button
    int       m_buttonState;

    ///< Padding on each side of the bitmap
    int       m_padding;

    ///< Size without the padding
    wxSize    m_unadjustedMinSize;

    ///< Accept mouse-up as click even if mouse-down happened outside of the control
    bool      m_acceptDraggedInClicks;
};

#endif /*BITMAP_BUTTON_H_*/
