/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#include "wx/panel.h"

#include <vector>
#include <functional>


// Forward defs for private-only classes
class wxBoxSizer;


/**
 * A panel that contains buttons, arranged on the left and/or right sides.
 */
class BUTTON_ROW_PANEL: public wxPanel
{
public:

    /**
     * Callback function definition. A callback of this type can be registered
     * to handle the button click event.
     */
    using BTN_CALLBACK = std::function< void( wxCommandEvent& ) >;

    /**
     * The information needed to instantiate a button on a BUTTON_ROW_PANEL.
     */
    struct BTN_DEF
    {
        /**
         * The button ID. Can be wxID_ANY, but should be unique if you
         * want to work out which button this was from an event handler.
         */
        wxWindowID      m_id;

        /**
         * The button display text.
         */
        wxString        m_text;

        /**
         * Button tooltip text - empty string for no tooltip
         */
        wxString        m_tooltip;

        /**
         * The callback fired when the button is clicked. Can be nullptr,
         * but then the button is useless.
         */
        BTN_CALLBACK    m_callback;
    };

    /**
     * A list of BTN_DEFs, used to group buttons into the left/right groups.
     */
    using BTN_DEF_LIST = std::vector<BTN_DEF>;

    /**
     * Construct a SIMPLE_BUTTON_PANEL with a set of buttons on each side.
     *
     * @param aLeftBtns: buttons on the left side, from left to right
     * @param aRightBtns: buttons on the right side, from left to right
     */
    BUTTON_ROW_PANEL( wxWindow* aWindow, const BTN_DEF_LIST& aLeftBtns, const BTN_DEF_LIST& aRightBtns );

private:
    /**
     * Add a set of buttons to one side of the panel.
     *
     * @param aSizer the sizer to add them to
     * @param aLeft  place on the left (false for right)
     * @param aDefs  list of button defs, from left to right
     */
    void addButtons( bool aLeft, const BTN_DEF_LIST& aDefs );

private:
    wxBoxSizer* m_sizer;
};
