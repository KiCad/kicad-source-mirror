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

#include <kicommon.h>
#include <wx/statbmp.h>
#include <wx/panel.h>

class wxStaticBitmap;

/**
 * Represent a row indicator icon for use in places like the layer widget.
 */
class KICOMMON_API INDICATOR_ICON : public wxPanel
{
public:
    /**
     * An id that refers to a certain icon state.
     *
     * Exactly what that state might mean in terms of icons is up to the icon provider.
     */
    using ICON_ID = int;

    /**
     * A simple object that can provide fixed bitmaps for use as row indicators.
     */
    class ICON_PROVIDER
    {
    public:
        virtual ~ICON_PROVIDER() {};

        /**
         * Get a reference to the row icon in the given mode.
         *
         * @param aIconId the id of the icon to get (depends on the provider).
         */
        virtual const wxBitmap& GetIndicatorIcon( ICON_ID aIconId ) const = 0;
    };

    /**
     * @param aParent the owning window.
     * @param aIconProvider the icon provider to get icons from.
     * @param aInitialIcon is the initial state of the icon.
     * @param aID the ID to use for the widgets - events will have this ID.
     */
    INDICATOR_ICON( wxWindow* aParent, ICON_PROVIDER& aIconProvider, ICON_ID aInitialIcon, int aID );

    /**
     * Set the row indicator to the given state.
     *
     * @param aIconId the icon ID to pass to the provider.
     */
    void SetIndicatorState( ICON_ID aIconId );

    /**
     * Update the window ID of this control and its children.
     *
     * @param aId new Window ID to set.
     */
    void SetWindowID( wxWindowID aId )
    {
        SetId( aId );
        m_bitmap->SetId( aId );
    }

private:
    ICON_PROVIDER&   m_iconProvider;   /// Object that delivers icons for the indicator
    wxStaticBitmap*  m_bitmap;         /// Handle on the bitmap widget.
    ICON_ID          m_currentId;      /// Is the icon currently "on".
};


/**
 * Icon provider for the "standard" row indicators, for example in layer selection lists
 */
class KICOMMON_API ROW_ICON_PROVIDER : public INDICATOR_ICON::ICON_PROVIDER
{
public:

    /// State constants to select the right icons
    enum STATE
    {
        OFF,    ///< Row "off" or "deselected"
        DIMMED, ///< Row "dimmed"
        ON,     ///< Row "on" or "selected"
        UP,     ///< Row above design alpha
        DOWN,   ///< Row below design alpha
        OPEN,   ///< Tree control open
        CLOSED  ///< Tree control closed
    };

    ROW_ICON_PROVIDER( int aSizeDIP, wxWindow* aWindow );

    /// @copydoc INDICATOR_ICON::ICON_PROVIDER::GetIndicatorIcon()
    const wxBitmap& GetIndicatorIcon( INDICATOR_ICON::ICON_ID aIconId ) const override;

private:
    wxBitmap m_blankBitmap;
    wxBitmap m_rightArrowBitmap;
    wxBitmap m_upArrowBitmap;
    wxBitmap m_downArrowBitmap;
    wxBitmap m_dotBitmap;
    wxBitmap m_openBitmap;
    wxBitmap m_closedBitmap;
};
