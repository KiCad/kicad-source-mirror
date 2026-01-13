/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <wx/bitmap.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/statbmp.h>
#include <wx/window.h>

#include <functional>

#include <gal/color4d.h>
#include <dialogs/dialog_color_picker.h>

enum SWATCH_SIZE
{
    SWATCH_SMALL,
    SWATCH_MEDIUM,
    SWATCH_LARGE,
    SWATCH_EXPAND
};


const static wxSize SWATCH_SIZE_SMALL_DU( 8, 6 );
const static wxSize SWATCH_SIZE_MEDIUM_DU( 24, 10 );
const static wxSize SWATCH_SIZE_LARGE_DU( 24, 16 );
const static wxSize CHECKERBOARD_SIZE_DU( 3, 3 );


/**
 * A simple color swatch of the kind used to set layer colors.
 */
class COLOR_SWATCH: public wxPanel
{
public:

    /**
     * Construct a COLOR_SWATCH.
     *
     * @param aParent parent window.
     * @param aColor initial swatch color.
     * @param aID id to use when sending swatch events.
     */
    COLOR_SWATCH( wxWindow* aParent, const KIGFX::COLOR4D& aColor, int aID,
                  const KIGFX::COLOR4D& aBackground, const KIGFX::COLOR4D& aDefault,
                  SWATCH_SIZE aSwatchType, bool aTriggerWithSingleClick = false );

    /**
     * Constructor for wxFormBuilder.
     */
    COLOR_SWATCH( wxWindow *aParent, wxWindowID aId, const wxPoint &aPos = wxDefaultPosition,
                  const wxSize &aSize = wxDefaultSize, long aStyle = 0 );

    /**
     * Set the current swatch color directly.
     */
    void SetSwatchColor( const KIGFX::COLOR4D& aColor, bool aSendEvent );

    /**
     * Sets the color that will be chosen with the "Reset to Default" button in the chooser
     */
    void SetDefaultColor( const KIGFX::COLOR4D& aColor );

    /**
     * Set the swatch background color.
     */
    void SetSwatchBackground( const KIGFX::COLOR4D& aBackground );

    /**
     * @return the current swatch color.
     */
    KIGFX::COLOR4D GetSwatchColor() const;

    /**
     * Update the window ID of this control and its children.
     *
     * @param aId new Window ID to set.
     */
    void SetWindowID( wxWindowID aId )
    {
        SetId( aId );
        m_swatch->SetId( aId );
    }

    /**
     * Prompt for a new colour, using the colour picker dialog.
     *
     * A colour change event will be sent if it's set.
     */
    void GetNewSwatchColor();

    void SetReadOnly( bool aReadOnly = true ) { m_readOnly = aReadOnly; }
    bool IsReadOnly() const { return m_readOnly; }

    void SetSupportsOpacity( bool aSupportsOpacity ) { m_supportsOpacity = aSupportsOpacity; }

    /// Register a handler for when the user tries to interact with a read-only swatch.
    void SetReadOnlyCallback( std::function<void()> aCallback ) { m_readOnlyCallback = aCallback; }

    /// Respond to a change in the OS's DarkMode setting.
    void OnDarkModeToggle();

    static wxBitmap MakeBitmap( const KIGFX::COLOR4D& aColor, const KIGFX::COLOR4D& aBackground,
                                const wxSize& aSize, const wxSize& aCheckerboardSize,
                                const KIGFX::COLOR4D& aCheckerboardBackground,
                                const std::vector<int>& aMargins = { 0, 0, 0, 0 } );

    static void RenderToDC( wxDC* aDC, const KIGFX::COLOR4D& aColor, const KIGFX::COLOR4D& aBackground,
                            const wxRect& aRect, const wxSize& aCheckerboardSize,
                            const KIGFX::COLOR4D& aCheckerboardBackground,
                            const std::vector<int>& aMargins = { 0, 0, 0, 0 } );

private:
    void setupEvents( bool aTriggerWithSingleClick );

    wxBitmap makeBitmap();

    /**
     * Pass unwanted events on to listeners of this object.
     */
    void rePostEvent( wxEvent& aEvent );

    /**
     * Handle mouse events on the swatch, and trigger the color picker dialog if appropriate.
     * Binds to the event sink so it is properly freed when the swatch is destroyed.
     */
    void onMouseEvent( wxEvent& aEvent );

    KIGFX::COLOR4D        m_color;
    KIGFX::COLOR4D        m_background;
    KIGFX::COLOR4D        m_default;
    CUSTOM_COLORS_LIST*   m_userColors;

    wxStaticBitmap*       m_swatch;

    wxSize                m_size;
    wxSize                m_checkerboardSize;
    KIGFX::COLOR4D        m_checkerboardBg;

    /// A read-only swatch won't show the color chooser dialog but otherwise works normally
    bool                  m_readOnly;
    std::function<void()> m_readOnlyCallback;

    /// If opacity is not supported the color chooser dialog will be displayed without it
    bool                  m_supportsOpacity;
};


/**
 * Event signaling a swatch has changed color
 */
wxDECLARE_EVENT( COLOR_SWATCH_CHANGED, wxCommandEvent );
