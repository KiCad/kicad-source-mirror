/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FOOTPRINT_DIFF_WIDGET_H
#define FOOTPRINT_DIFF_WIDGET_H

#include <widgets/footprint_preview_widget.h>


class FOOTPRINT;
class wxBitmapButton;
class wxSlider;


class FOOTPRINT_DIFF_WIDGET: public FOOTPRINT_PREVIEW_WIDGET
{
public:
    /**
     * Construct a footprint diff widget, consisting on a canvas for displaying a board and
     * a library footprint, and a slider for fading between the two.
     *
     * @param aParent - parent window
     * @param aKiway - an active Kiway instance
     */
    FOOTPRINT_DIFF_WIDGET( wxWindow* aParent, KIWAY& aKiway );

    /**
     * Set the currently displayed symbol.
     */
    void DisplayDiff( FOOTPRINT* aBoardFootprint, std::shared_ptr<FOOTPRINT>& aLibFootprint );

    /**
     * Toggle between full-A and full-B display.
     */
    void ToggleAB();

private:
    void onSlider( wxScrollEvent& aEvent );
    void onCharHook( wxKeyEvent& aEvent );

private:
    std::shared_ptr<FOOTPRINT> m_boardItemCopy;
    std::shared_ptr<FOOTPRINT> m_libraryItem;
    wxSlider*                  m_slider;
    wxBitmapButton*            m_toggleButton;
};


#endif // FOOTPRINT_DIFF_WIDGET_H
