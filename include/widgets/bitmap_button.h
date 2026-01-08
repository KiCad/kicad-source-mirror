/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee dot org>
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

#ifndef BITMAP_BUTTON_H_
#define BITMAP_BUTTON_H_

#include <kicommon.h>
#include <wx/bmpbndl.h>
#include <wx/panel.h>
#include <wx/colour.h>


/**
 * A bitmap button widget that behaves like an AUI toolbar item's button when it is drawn.
 * Specifically:
 *     * It has no border
 *     * It has a rectangle highlight when the mouse is hovering/pressed
 *     * It has the ability to be checked/toggled
 */
class KICOMMON_API BITMAP_BUTTON : public wxPanel
{
public:
    BITMAP_BUTTON( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos = wxDefaultPosition,
                   const wxSize& aSize = wxDefaultSize,
                   int aStyles = wxBORDER_NONE | wxTAB_TRAVERSAL );

    // For use with wxFormBuilder on a sub-classed wxBitmapButton
    BITMAP_BUTTON( wxWindow* aParent, wxWindowID aId, const wxBitmap& aDummyBitmap,
                   const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                   int aStyles = wxBORDER_NONE | wxTAB_TRAVERSAL );

    ~BITMAP_BUTTON();

    /**
     * Set the amount of padding present on each side of the bitmap.
     *
     * @param aPadding is the amount in DIP of padding for each side.
     */
    void SetPadding( int aPaddingDIP );

    /**
     * Set the bitmap shown when the button is enabled.
     *
     * @param aBmp is the enabled bitmap.
     */
    void SetBitmap( const wxBitmapBundle& aBmp );

    /**
     * Set the bitmap shown when the button is disabled.
     *
     * @param aBmp is the disabled bitmap.
     */
    void SetDisabledBitmap( const wxBitmapBundle& aBmp );

    /**
     * Enable the button.
     */
    bool Enable( bool aEnable = true ) override;

    /**
     * Setup the control as a two-state button (checked or unchecked).
     */
    void SetIsCheckButton();

    void SetIsRadioButton();

    /**
     * Check the control. This is the equivalent to toggling a toolbar button.
     */
    void Check( bool aCheck = true );

    bool IsChecked() const;

    /**
     * Render button as a toolbar separator.
     *
     * Also disables the button.  Bitmap, if set, is ignored.
     */
    void SetIsSeparator();

    /**
     * Accept mouse-up as click even if mouse-down happened outside of the control
     *
     * @param aAcceptDragIn is true to allow drag in, false to ignore lone mouse-up events
     */
    void AcceptDragInAsClick( bool aAcceptDragIn = true );

    void SetShowBadge( bool aShowBadge ) { m_showBadge = aShowBadge; }

    void SetBadgeText( const wxString& aText ) { m_badgeText = aText; }

    void SetBadgeColors( const wxColor& aBadgeColor, const wxColor& aBadgeTextColor )
    {
        m_badgeColor = aBadgeColor;
        m_badgeTextColor = aBadgeTextColor;
    }

    void SetBitmapCentered( bool aCentered = true )
    {
        m_centerBitmap = aCentered;
    }

    void SetIsToolbarButton( bool aIsToolbar = true ) { m_isToolbarButton = aIsToolbar; }
    bool IsToolbarButton() const { return m_isToolbarButton; }

protected:
    void setupEvents();

    void OnMouseLeave( wxEvent& aEvent );
    void OnMouseEnter( wxEvent& aEvent );
    void OnKillFocus( wxEvent& aEvent );
    void OnSetFocus( wxEvent& aEvent );
    void OnLeftButtonUp( wxMouseEvent& aEvent );
    void OnLeftButtonDown( wxMouseEvent& aEvent );
    void OnPaint( wxPaintEvent& aEvent );
    void OnDPIChanged( wxDPIChangedEvent& aEvent );

    virtual wxSize DoGetBestSize() const override;

    void setFlag( int aFlag )
    {
        m_buttonState |= aFlag;
    }

    void clearFlag( int aFlag )
    {
        m_buttonState &= ~aFlag;
    }

    bool hasFlag( int aFlag ) const
    {
        return m_buttonState & aFlag;
    }

    void invalidateBestSize();

private:
    wxBitmapBundle m_normalBitmap;
    wxBitmapBundle m_disabledBitmap;

    bool      m_isRadioButton;
    bool      m_showBadge;
    wxString  m_badgeText;
    wxColor   m_badgeColor;
    wxColor   m_badgeTextColor;
    wxFont    m_badgeFont;
    int       m_buttonState;
    int       m_padding;
    wxSize    m_dipSize;
    bool      m_isToolbarButton;

    /// Accept mouse-up as click even if mouse-down happened outside of the control.
    bool      m_acceptDraggedInClicks;

    /// Draw bitmap centered in the control.
    bool      m_centerBitmap;
};

#endif /*BITMAP_BUTTON_H_*/
