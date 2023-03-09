/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SYMBOL_DIFF_WIDGET_H
#define SYMBOL_DIFF_WIDGET_H

#include <wx/panel.h>
#include <kiway.h>
#include <gal/gal_display_options.h>
#include <widgets/symbol_preview_widget.h>


class LIB_SYMBOL;
class wxSlider;


class SYMBOL_DIFF_WIDGET: public SYMBOL_PREVIEW_WIDGET
{
public:
    /**
     * Construct a symbol diff widget.
     *
     * @param aParent - parent window
     * @param aCanvasType = the type of canvas (GAL_TYPE_OPENGL or GAL_TYPE_CAIRO only)
     */
    SYMBOL_DIFF_WIDGET( wxWindow* aParent, EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    ~SYMBOL_DIFF_WIDGET() override;

    /**
     * Set the currently displayed symbol.
     */
    void DisplayDiff( LIB_SYMBOL* aSchSymbol, LIB_SYMBOL* aLibSymbol, int aUnit, int aConvert );

private:
    void onSlider( wxScrollEvent& aEvent );

private:
    LIB_SYMBOL* m_libraryItem;
    wxSlider*   m_slider;
};


#endif // SYMBOL_DIFF_WIDGET_H
