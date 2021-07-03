/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef WIDGETS_GAL_OPTIONS_PANEL__H_
#define WIDGETS_GAL_OPTIONS_PANEL__H_

#include <wx/panel.h>

#include <gal/gal_display_options.h>

class wxBoxSizer;
class wxRadioBox;
class wxSpinCtrlDouble;
class wxStaticText;
class EDA_DRAW_FRAME;

class GAL_OPTIONS_PANEL: public wxPanel
{
public:

    GAL_OPTIONS_PANEL( wxWindow* aParent, EDA_DRAW_FRAME* aDrawFrame );

    /**
     * Load the panel controls from the given opt
     */
    bool TransferDataToWindow() override;

    /**
     * Read the options set in the UI into the given options object
     */
    bool TransferDataFromWindow() override;

private:
    EDA_DRAW_FRAME*   m_drawFrame;

    wxBoxSizer*       m_mainSizer;

#ifndef __WXMAC__
    wxRadioBox*       m_renderingEngine;
#endif

    wxRadioBox*       m_gridStyle;
    wxStaticText*     l_gridLineWidth;
    wxSpinCtrlDouble* m_gridLineWidth;
    wxStaticText*     l_gridLineWidthUnits;

    wxStaticText*     l_gridMinSpacing;
    wxSpinCtrlDouble* m_gridMinSpacing;
    wxStaticText*     l_gridMinSpacingUnits;

    wxStaticText*     l_gridSnapOptions;
    wxChoice*         m_gridSnapOptions;
    wxStaticText*     l_gridSnapSpace;

    wxRadioBox*       m_cursorShape;
    wxCheckBox*       m_forceCursorDisplay;

    ///< The GAL options to read/write
    KIGFX::GAL_DISPLAY_OPTIONS& m_galOptions;
};


#endif // WIDGETS_GAL_OPTIONS_PANEL__H_
